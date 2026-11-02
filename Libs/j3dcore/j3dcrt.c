// Microsoft CRT function shim for non-Windows (Null platform), following Microsoft's documented
// behavior so engine code compiles unchanged against the Windows secure CRT. Windows never
// compiles this file (j3dcore builds it into the j3dcrt static library on J3D_PLATFORM_NULL only).
// The test coverage lives in Tests/j3dCrtTest.c.
#ifndef _WIN32

#include "j3dcrt.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include <dirent.h>
#include <sys/stat.h>

// Added: j3dcrt.h routes every engine fopen through j3dcrt_fopen below, so this file names the
// C runtime entry point directly instead of the macro while implementing it.
#undef fopen

#if defined(__GNUC__)
// The scanned formats are built internally from recovered engine format strings.
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

// Mirrors Microsoft's strnlen_s (<corecrt_string.h>): length of the string, capped at maxsize,
// and 0 for a null pointer.
size_t strnlen_s(const char* pString, size_t maxsize)
{
    size_t numChars = 0;

    if ( pString )
    {
        while ( numChars < maxsize && pString[numChars] )
        {
            ++numChars;
        }
    }

    return numChars;
}

// Mirrors Microsoft's strcpy_s: 0 on success, EINVAL for null destinations or a zero size, and
// ERANGE with the destination emptied when the source does not fit.
errno_t strcpy_s(char* pDest, rsize_t destSize, const char* pSource)
{
    if ( !pDest || destSize == 0 )
    {
        return EINVAL;
    }
    if ( !pSource )
    {
        pDest[0] = '\0';
        return EINVAL;
    }

    size_t numChars = strnlen_s(pSource, destSize);
    if ( numChars >= destSize )
    {
        pDest[0] = '\0';
        errno = ERANGE;
        return ERANGE;
    }

    memcpy(pDest, pSource, numChars + 1);
    return 0;
}

// Mirrors Microsoft's strncpy_s: copies up to count characters or the source terminator, always
// terminates, returns STRUNCATE when count is _TRUNCATE and the source is truncated, and
// ERANGE with the destination emptied when the requested copy does not fit.
errno_t strncpy_s(char* pDest, rsize_t destSize, const char* pSource, size_t count)
{
    if ( !pDest || destSize == 0 )
    {
        return EINVAL;
    }

    size_t copyLen;
    if ( !pSource )
    {
        pDest[0] = '\0';
        return EINVAL;
    }
    if ( count == _TRUNCATE )
    {
        copyLen = strnlen_s(pSource, destSize);
        if ( copyLen >= destSize )
        {
            memcpy(pDest, pSource, destSize - 1);
            pDest[destSize - 1] = '\0';
            return STRUNCATE;
        }
        memcpy(pDest, pSource, copyLen + 1);
        return 0;
    }
    if ( count == 0 )
    {
        pDest[0] = '\0';
        return 0;
    }

    copyLen = strnlen_s(pSource, count);
    if ( copyLen >= destSize )
    {
        pDest[0] = '\0';
        errno = ERANGE;
        return ERANGE;
    }

    memcpy(pDest, pSource, copyLen);
    pDest[copyLen] = '\0';
    return 0;
}

// Wide-character equivalent of j3dcrt_WcsLen: length of the wide string, capped at maxsize.
static size_t j3dcrt_Wcsnlen(const wchar_t* pString, size_t maxsize)
{
    size_t numChars = 0;

    if ( pString )
    {
        while ( numChars < maxsize && pString[numChars] )
        {
            ++numChars;
        }
    }

    return numChars;
}

// Mirrors Microsoft's wcsncpy_s: wide version of strncpy_s with the same truncation and error
// semantics.
errno_t wcsncpy_s(wchar_t* pDest, rsize_t destSize, const wchar_t* pSource, size_t count)
{
    if ( !pDest || destSize == 0 )
    {
        return EINVAL;
    }

    if ( !pSource )
    {
        pDest[0] = L'\0';
        return EINVAL;
    }

    size_t copyLen;
    if ( count == _TRUNCATE )
    {
        copyLen = j3dcrt_Wcsnlen(pSource, destSize);
        if ( copyLen >= destSize )
        {
            memcpy(pDest, pSource, (destSize - 1) * sizeof(wchar_t));
            pDest[destSize - 1] = L'\0';
            return STRUNCATE;
        }
        memcpy(pDest, pSource, (copyLen + 1) * sizeof(wchar_t));
        return 0;
    }
    if ( count == 0 )
    {
        pDest[0] = L'\0';
        return 0;
    }

    copyLen = j3dcrt_Wcsnlen(pSource, count);
    if ( copyLen >= destSize )
    {
        pDest[0] = L'\0';
        errno = ERANGE;
        return ERANGE;
    }

    memcpy(pDest, pSource, copyLen * sizeof(wchar_t));
    pDest[copyLen] = L'\0';
    return 0;
}

// Mirrors Microsoft's strncat_s: appends up to count source characters to the destination,
// terminates, returns STRUNCATE for _TRUNCATE truncation, and reports EINVAL for null arguments,
// a zero size or an unterminated destination, ERANGE (destination emptied) when it does not fit.
errno_t strncat_s(char* pDest, rsize_t destSize, const char* pSource, size_t count)
{
    if ( !pDest || destSize == 0 )
    {
        return EINVAL;
    }

    if ( !pSource )
    {
        pDest[0] = '\0';
        return EINVAL;
    }

    size_t destLen = strnlen_s(pDest, destSize);
    if ( destLen >= destSize )
    {
        pDest[0] = '\0';
        return EINVAL;
    }

    size_t copyLen;
    if ( count == _TRUNCATE )
    {
        copyLen = strnlen_s(pSource, destSize - destLen);
        if ( copyLen > destSize - destLen - 1 )
        {
            copyLen = destSize - destLen - 1;
            memcpy(pDest + destLen, pSource, copyLen);
            pDest[destLen + copyLen] = '\0';
            return STRUNCATE;
        }
    }
    else
    {
        copyLen = strnlen_s(pSource, count);
        if ( copyLen > destSize - destLen - 1 )
        {
            pDest[0] = '\0';
            errno = ERANGE;
            return ERANGE;
        }
    }

    memcpy(pDest + destLen, pSource, copyLen);
    pDest[destLen + copyLen] = '\0';
    return 0;
}

int j3dcrt_PrintS(char* pBuf, size_t bufSize, size_t count, const char* pFormat, va_list args)
{
    if ( !pBuf || bufSize == 0 || !pFormat )
    {
        // Microsoft: the buffer is emptied and -1 returned for the invalid arguments.
        if ( pBuf && bufSize != 0 )
        {
            pBuf[0] = '\0';
        }
        errno = EINVAL;
        return -1;
    }
    if ( count == 0 )
    {
        // Microsoft: count == 0 places a terminator at the start and returns -1.
        pBuf[0] = '\0';
        return -1;
    }
    if ( count == _TRUNCATE || count < bufSize )
    {
        size_t writeMax = ( count < bufSize ) ? count + 1 : bufSize;
        int numWritten = vsnprintf(pBuf, writeMax, pFormat, args);
        if ( numWritten < 0 || (size_t)numWritten >= writeMax )
        {
            return -1;
        }
        return numWritten;
    }
    // count >= bufSize and no truncation: Microsoft formats into the buffer, and clears it and
    // reports ERANGE only when the output does not fit.
    int numWritten = vsnprintf(pBuf, bufSize, pFormat, args);
    if ( numWritten < 0 )
    {
        return -1;
    }
    if ( (size_t)numWritten >= bufSize )
    {
        pBuf[0] = '\0';
        errno = ERANGE;
        return -1;
    }
    return numWritten;
}

int j3dcrt_WPrintS(wchar_t* pBuf, size_t bufSize, size_t count, const wchar_t* pFormat, va_list args)
{
    if ( !pBuf || bufSize == 0 || !pFormat )
    {
        if ( pBuf && bufSize != 0 )
        {
            pBuf[0] = L'\0';
        }
        errno = EINVAL;
        return -1;
    }
    if ( count == 0 )
    {
        // Microsoft: count == 0 places a terminator at the start and returns -1.
        pBuf[0] = L'\0';
        return -1;
    }
    if ( count == _TRUNCATE || count < bufSize )
    {
        size_t writeMax = ( count < bufSize ) ? count + 1 : bufSize;
        int numWritten = vswprintf(pBuf, writeMax, pFormat, args);
        if ( numWritten < 0 || (size_t)numWritten >= writeMax )
        {
            return -1;
        }
        return numWritten;
    }
    int numWritten = vswprintf(pBuf, bufSize, pFormat, args);
    if ( numWritten < 0 )
    {
        return -1;
    }
    if ( (size_t)numWritten >= bufSize )
    {
        pBuf[0] = L'\0';
        errno = ERANGE;
        return -1;
    }
    return numWritten;
}

// Mirrors Microsoft's vsnprintf_s (<stdio.h>-documented set with _snprintf_s): with _TRUNCATE,
// truncates and returns -1 when the output does not fit, else the length; with count < bufSize,
// writes at most count characters plus a terminator and returns -1 when truncated; otherwise
// clears buffer[0], sets ERANGE and returns -1 when the output does not fit.
int vsnprintf_s(char* pBuf, size_t bufSize, size_t count, const char* pFormat, va_list args)
{
    return j3dcrt_PrintS(pBuf, bufSize, count, pFormat, args);
}

// Variadic front end of vsnprintf_s with the same Microsoft behavior.
int _snprintf_s(char* pBuf, size_t bufSize, size_t count, const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    int ret = j3dcrt_PrintS(pBuf, bufSize, count, pFormat, args);
    va_end(args);
    return ret;
}

// Mirrors Microsoft's sprintf_s: like _snprintf_s without truncation; a too-small buffer is
// empty by clearing buffer[0] and returns -1 (errno ERANGE).
int sprintf_s(char* pBuf, size_t bufSize, const char* pFormat, ...)
{
    if ( !pBuf || bufSize == 0 || !pFormat )
    {
        if ( pBuf && bufSize != 0 )
        {
            pBuf[0] = '\0';
        }
        errno = EINVAL;
        return -1;
    }

    va_list args;
    va_start(args, pFormat);

    int numWritten = vsnprintf(pBuf, bufSize, pFormat, args);
    va_end(args);
    if ( numWritten < 0 )
    {
        return -1;
    }
    if ( (size_t)numWritten >= bufSize )
    {
        pBuf[0] = '\0';
        errno = ERANGE;
        return -1;
    }
    return numWritten;
}

// Mirrors Microsoft's _vsnwprintf_s: the document-internally wide equivalent of _snprintf_s as
// described above, built on vswprintf.
int _vsnwprintf_s(wchar_t* pBuf, size_t bufSize, size_t count, const wchar_t* pFormat, va_list args)
{
    return j3dcrt_WPrintS(pBuf, bufSize, count, pFormat, args);
}

// Added: the path size the resolver writes, matching the Windows MAX_PATH limit the engine's path
// buffers are sized for. A longer path is passed to the C runtime unresolved.
#define J3DCRT_MAXPATH 260

// Added: the longest directory entry name the resolver matches, and the search order it picks from:
// case-insensitive name order with the byte order breaking ties, so the choice does not depend on
// the order the directory is read in.
#define J3DCRT_MAXNAME 256

// Added: looks up one path component in an existing directory: reports the first entry whose name
// matches pName case-insensitively in the order above through pNameOut. Returns false when the
// directory can not be opened or holds no such entry.
static bool j3dcrt_FindPathEntry(const char* pDirPath, const char* pName, char* pNameOut, size_t nameSize)
{
    DIR* pDir = opendir(pDirPath);
    if ( !pDir )
    {
        return false;
    }

    bool           bMatched = false;
    struct dirent* pEntry;
    while ( ( pEntry = readdir(pDir) ) != NULL )
    {
        if ( strcasecmp(pEntry->d_name, pName) != 0 || strlen(pEntry->d_name) >= nameSize )
        {
            continue;
        }

        if ( !bMatched || strcasecmp(pEntry->d_name, pNameOut) < 0 ||
            ( strcasecmp(pEntry->d_name, pNameOut) == 0 && strcmp(pEntry->d_name, pNameOut) < 0 ) )
        {
            strcpy_s(pNameOut, nameSize, pEntry->d_name);
            bMatched = true;
        }
    }

    closedir(pDir);
    return bMatched;
}

// Added: Windows paths are case-insensitive and use backslashes. Writes the real path of pPath to
// pOut, truncating to size bytes: backslashes become slashes and every component that does not
// exist with the case it is written in is replaced by the first case-insensitive match in its
// directory. Components that match nothing, and everything after them, stay as given so a file that
// does not exist yet can still be created. Returns pOut.
char* j3dcrt_ResolvePath(const char* pPath, char* pOut, size_t size)
{
    struct stat st;

    if ( !pOut || size == 0 )
    {
        return pOut;
    }

    pOut[0] = '\0';
    if ( !pPath )
    {
        return pOut;
    }

    size_t outLen    = 0;
    size_t cur       = 0;
    bool   bAbsolute = false;
    if ( pPath[0] == '/' )
    {
        // Added: a leading slash is absolute on both platforms and resolves from the root.
        bAbsolute = true;
        cur       = 1;
        pOut[outLen] = '/';
        ++outLen;
    }
    else if ( pPath[0] == '\\' )
    {
        // Added: a leading backslash is a Windows drive-relative path. There is no drive to root it
        // against here, so it resolves against the current directory like a plain relative path.
        cur = 1;
    }

    while ( pPath[cur] && outLen + 1 < size )
    {
        pOut[outLen] = ( pPath[cur] == '\\' ) ? '/' : pPath[cur];
        ++cur;
        ++outLen;
    }
    pOut[outLen] = '\0';

    size_t compStart = ( bAbsolute ) ? 1 : 0;
    while ( compStart < outLen )
    {
        size_t compEnd = compStart;
        while ( compEnd < outLen && pOut[compEnd] != '/' )
        {
            ++compEnd;
        }

        char aComp[J3DCRT_MAXNAME];
        size_t compLen = compEnd - compStart;
        if ( compLen == 0 || compLen >= sizeof(aComp) )
        {
            // An empty component ("//" or a trailing slash) and an unmatchably long name stay as given.
            break;
        }

        memcpy(aComp, pOut + compStart, compLen);
        aComp[compLen] = '\0';

        // A component that exists with the case it was written in is already the real name.
        char chTail   = pOut[compEnd];
        pOut[compEnd] = '\0';
        bool bExact   = ( stat(pOut, &st) == 0 );
        pOut[compEnd] = chTail;
        if ( bExact )
        {
            compStart = compEnd + 1;
            continue;
        }

        char aMatch[J3DCRT_MAXNAME];
        bool bFound;
        if ( compStart == 0 )
        {
            bFound = j3dcrt_FindPathEntry(".", aComp, aMatch, sizeof(aMatch));
        }
        else
        {
            char chFirst    = pOut[compStart];
            pOut[compStart] = '\0';
            bFound          = j3dcrt_FindPathEntry(pOut, aComp, aMatch, sizeof(aMatch));
            pOut[compStart] = chFirst;
        }
        if ( !bFound )
        {
            break;
        }

        size_t matchLen = strlen(aMatch);
        size_t newLen   = outLen - compLen + matchLen;
        if ( newLen + 1 > size )
        {
            // The real name does not fit the caller's buffer, so the path stays as given.
            break;
        }

        memmove(pOut + compStart + matchLen, pOut + compEnd, outLen - compEnd + 1);
        memcpy(pOut + compStart, aMatch, matchLen);
        outLen    = newLen;
        compStart = compStart + matchLen + 1;
    }

    return pOut;
}

// Added: fopen on the resolved path, so the engine's backslash and mixed-case path spellings open
// the real file the way they do on Windows. Returns the FILE*, or NULL with errno set when the file
// can not be opened. A path longer than J3DCRT_MAXPATH is used unresolved.
FILE* j3dcrt_fopen(const char* pName, const char* pMode)
{
    char aPath[J3DCRT_MAXPATH];

    if ( !pName )
    {
        errno = EINVAL;
        return NULL;
    }

    if ( strlen(pName) >= sizeof(aPath) )
    {
        return fopen(pName, pMode);
    }

    j3dcrt_ResolvePath(pName, aPath, sizeof(aPath));
    return fopen(aPath, pMode);
}

// Mirrors Microsoft's fopen_s: opens pFilename with j3dcrt_fopen, stores null in *ppFile and returns
// the errno value when it fails, and 0 on success.
errno_t fopen_s(FILE** ppFile, const char* pFilename, const char* pMode)
{
    if ( !ppFile )
    {
        return EINVAL;
    }

    *ppFile = NULL;
    if ( !pFilename || !pMode )
    {
        return EINVAL;
    }

    *ppFile = j3dcrt_fopen(pFilename, pMode);
    if ( !*ppFile )
    {
        return errno ? errno : EIO;
    }
    return 0;
}

// scanf_s processing collects up to this many receiving arguments from the va_list.
#define J3DCRT_MAXSCANFARGS 32

typedef struct j3dcrt_ScanfCall
{
    // Receiving pointer of each active (non-silent) conversion, in format order.
    void* aPointers[J3DCRT_MAXSCANFARGS];
    // Buffer size argument that follows the pointer of %s, %c and %[ conversions.
    rsize_t aSizes[J3DCRT_MAXSCANFARGS];
    // Conversion character of each entry ('s', 'c', '['; numbers keep their own kind).
    char aKinds[J3DCRT_MAXSCANFARGS];
    // Assignment index of each entry, counted over the converting (non-%n) conversions.
    int aCheckPositions[J3DCRT_MAXSCANFARGS];
    // Scan buffer used instead of the caller's buffer for %s and %[ conversions whose field
    // width can fill it entirely, so the terminator never writes past the caller's size. NULL
    // when the conversion writes directly into the caller's buffer.
    char* aTemps[J3DCRT_MAXSCANFARGS];
    int numArgs;
} j3dcrt_ScanfCall;

// Releases any temporary scan buffers kept for the conversion.
static void j3dcrt_FreeScanfTemps(j3dcrt_ScanfCall* pCall)
{
    for ( int cur = 0; cur < pCall->numArgs; ++cur )
    {
        free(pCall->aTemps[cur]);
        pCall->aTemps[cur] = NULL;
    }
}

// sscanf_s conversion walker: builds the rewritten format, inserting the buffer size as the
// width of each %s and %[ lacking its own width, and gathers pointers and sizes from args.
// Returns the rewritten format, or null when the format exceeds the supported conversions.
static char* j3dcrt_BuildScanfFormat(const char* pFormat, va_list args, j3dcrt_ScanfCall* pCall)
{
    size_t numSpecs = 0;
    const char* cur = pFormat;
    while ( *cur )
    {
        if ( *cur++ == '%' )
        {
            ++numSpecs;
        }
    }
    if ( numSpecs == 0 )
    {
        numSpecs = 1;
    }

    size_t allocSize = strlen(pFormat) * 21 + 2 * numSpecs + 2;
    char* pOut = malloc(allocSize);
    if ( !pOut )
    {
        return NULL;
    }

    char* pDst = pOut;
    char* pOutEnd = pOut + allocSize - 1;
    const char* pSrc = pFormat;
    int numConversions = 0;
    while ( *pSrc )
    {
        char curChar = *pSrc;
        if ( curChar != '%' )
        {
            *pDst++ = curChar;
            ++pSrc;
            continue;
        }

        char spec[64];
        int specLen = 0;
        bool bSilent = false;
        bool bHaveWidth = false;
        unsigned long width = 0;
        int widthEnd = 1;
        spec[specLen++] = '%';
        ++pSrc;
        if ( *pSrc == '%' )
        {
            spec[specLen++] = '%';
            ++pSrc;
        }
        else
        {
            if ( *pSrc == '*' )
            {
                bSilent = true;
                spec[specLen++] = '*';
                ++pSrc;
            }
            while ( isdigit((unsigned char)*pSrc) )
            {
                if ( width < 0x100000UL )
                {
                    width = width * 10UL + (unsigned long)( *pSrc - '0' );
                }
                spec[specLen++] = *pSrc++;
            }
            bHaveWidth = specLen > widthEnd;
            widthEnd = specLen;
            while ( *pSrc && strchr("hljztL", *pSrc) )
            {
                spec[specLen++] = *pSrc++;
            }
        }

        char convChar = *pSrc;
        if ( convChar )
        {
            ++pSrc;
        }

        if ( !bSilent && convChar == '\0' )
        {
            // Malformed specifier at end of format: copy what was written and stop.
            break;
        }

        if ( specLen + 2 > (int)sizeof(spec) )
        {
            free(pOut);
            return NULL;
        }
        if ( convChar )
        {
            spec[specLen++] = convChar;
        }

        if ( !bSilent && convChar )
        {
            void* pPoint = va_arg(args, void*);
            rsize_t size = ( convChar == 's' || convChar == 'c' || convChar == '[' )
                ? va_arg(args, rsize_t) : 1;
            if ( pCall->numArgs >= J3DCRT_MAXSCANFARGS )
            {
                free(pOut);
                return NULL;
            }
            pCall->aPointers[pCall->numArgs] = pPoint;
            pCall->aSizes[pCall->numArgs] = size;
            pCall->aKinds[pCall->numArgs] = convChar;
            pCall->aCheckPositions[pCall->numArgs] = numConversions;
            pCall->aTemps[pCall->numArgs] = NULL;
            ++pCall->numArgs;
            if ( convChar != 'n' )
            {
                ++numConversions;
            }

            // %s and %[ scan into a temporary buffer of width+1 bytes whenever the field can
            // fill the caller's buffer: scanning size or more characters into a width-sized
            // field would place sscanf's terminator at dest[size], one byte past the caller's
            // buffer. Conversions that keep their own (small enough) width write directly.
            bool bTempered = ( convChar == 's' || convChar == '[' )
                && ( !bHaveWidth || width >= (unsigned long)size );
            if ( bTempered )
            {
                size_t scanWidth = ( !bHaveWidth ) ? (size_t)size : (size_t)width;
                if ( scanWidth == 0 )
                {
                    // A zero size is Microsoft's failure case; still scan one character safely.
                    scanWidth = 1;
                }
                if ( scanWidth >= 0x100000UL )
                {
                    free(pOut);
                    return NULL;
                }
                pCall->aTemps[pCall->numArgs - 1] = malloc(scanWidth + 1);
                if ( !pCall->aTemps[pCall->numArgs - 1] )
                {
                    free(pOut);
                    return NULL;
                }
                pCall->aTemps[pCall->numArgs - 1][0] = '\0';

                if ( !bHaveWidth )
                {
                    char aWidth[24];
                    snprintf(aWidth, sizeof(aWidth), "%zu", scanWidth);
                    size_t numWidthChars = strlen(aWidth);
                    if ( (int)numWidthChars > (int)sizeof(spec) - specLen - 1 )
                    {
                        free(pOut);
                        return NULL;
                    }
                    memmove(spec + widthEnd + numWidthChars, spec + widthEnd,
                        (size_t)(specLen - widthEnd) + 1);
                    memcpy(spec + widthEnd, aWidth, numWidthChars);
                    specLen += (int)numWidthChars;
                }
            }
        }

        if ( pDst + specLen > pOutEnd )
        {
            free(pOut);
            return NULL;
        }
        memcpy(pDst, spec, (size_t)specLen);
        pDst += specLen;
    }
    *pDst = '\0';
    return pOut;
}

// Issues sscanf with the collected receiving pointers (excess converters are ignored).
static int j3dcrt_CallScanf(const char* pBuf, const char* pFormat, void* const* pPointers, int numArgs)
{
    switch ( numArgs )
    {
    case 0:  return sscanf(pBuf, pFormat);
    case 1:  return sscanf(pBuf, pFormat, pPointers[0]);
    case 2:  return sscanf(pBuf, pFormat, pPointers[0], pPointers[1]);
    case 3:  return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2]);
    case 4:  return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3]);
    case 5:  return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4]);
    case 6:  return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5]);
    case 7:  return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6]);
    case 8:  return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7]);
    case 9:  return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8]);
    case 10: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9]);
    case 11: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10]);
    case 12: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11]);
    case 13: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12]);
    case 14: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13]);
    case 15: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14]);
    case 16: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15]);
    case 17: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16]);
    case 18: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17]);
    case 19: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18]);
    case 20: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19]);
    case 21: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20]);
    case 22: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20], pPointers[21]);
    case 23: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20], pPointers[21], pPointers[22]);
    case 24: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20], pPointers[21], pPointers[22], pPointers[23]);
    case 25: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20], pPointers[21], pPointers[22], pPointers[23], pPointers[24]);
    case 26: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20], pPointers[21], pPointers[22], pPointers[23], pPointers[24], pPointers[25]);
    case 27: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20], pPointers[21], pPointers[22], pPointers[23], pPointers[24], pPointers[25], pPointers[26]);
    case 28: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20], pPointers[21], pPointers[22], pPointers[23], pPointers[24], pPointers[25], pPointers[26], pPointers[27]);
    case 29: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20], pPointers[21], pPointers[22], pPointers[23], pPointers[24], pPointers[25], pPointers[26], pPointers[27], pPointers[28]);
    case 30: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20], pPointers[21], pPointers[22], pPointers[23], pPointers[24], pPointers[25], pPointers[26], pPointers[27], pPointers[28], pPointers[29]);
    case 31: return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9], pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19], pPointers[20], pPointers[21], pPointers[22], pPointers[23], pPointers[24], pPointers[25], pPointers[26], pPointers[27], pPointers[28], pPointers[29], pPointers[30]);
    }
    return sscanf(pBuf, pFormat, pPointers[0], pPointers[1], pPointers[2], pPointers[3], pPointers[4], pPointers[5], pPointers[6], pPointers[7], pPointers[8], pPointers[9],
        pPointers[10], pPointers[11], pPointers[12], pPointers[13], pPointers[14], pPointers[15], pPointers[16], pPointers[17], pPointers[18], pPointers[19],
        pPointers[20], pPointers[21], pPointers[22], pPointers[23], pPointers[24], pPointers[25], pPointers[26], pPointers[27], pPointers[28], pPointers[29],
        pPointers[30], pPointers[31]);
}

// Mirrors Microsoft's vsscanf_s: %s, %c and %[ take an rsize_t buffer size after their pointer,
// %n and numbers do not, %* takes neither; a %s or %[ whose result fills its buffer without a
// terminator fails, empties the buffer and returns the number of conversions completed before it.
// Scans that could fill their buffer run into a temporary of width+1 bytes so sscanf's terminator
// never writes past the caller's size; the result is copied only when it fits.
int vsscanf_s(const char* pBuf, const char* pFormat, va_list args)
{
    j3dcrt_ScanfCall scan;
    scan.numArgs = 0;
    void* aArgs[J3DCRT_MAXSCANFARGS];

    char* pScanFormat = j3dcrt_BuildScanfFormat(pFormat, args, &scan);
    if ( !pScanFormat )
    {
        j3dcrt_FreeScanfTemps(&scan);
        return EOF;
    }

    for ( int cur = 0; cur < scan.numArgs; ++cur )
    {
        aArgs[cur] = ( scan.aTemps[cur] ) ? scan.aTemps[cur] : scan.aPointers[cur];
    }

    int numConverted = j3dcrt_CallScanf(pBuf, pScanFormat, aArgs, scan.numArgs);
    free(pScanFormat);

    int result = numConverted;
    if ( numConverted != EOF )
    {
        for ( int cur = 0; cur < scan.numArgs; ++cur )
        {
            if ( ( scan.aKinds[cur] == 's' || scan.aKinds[cur] == '[' ) && scan.aTemps[cur] && scan.aCheckPositions[cur] < numConverted )
            {
                char* pDest = (char*)scan.aPointers[cur];
                size_t numChars = strlen(scan.aTemps[cur]);
                if ( numChars >= (size_t)scan.aSizes[cur] )
                {
                    if ( scan.aSizes[cur] > 0 )
                    {
                        pDest[0] = '\0';
                    }
                    result = scan.aCheckPositions[cur];
                    break;
                }
                memcpy(pDest, scan.aTemps[cur], numChars + 1);
            }
        }
    }

    j3dcrt_FreeScanfTemps(&scan);
    return result;
}

// Variadic front end of vsscanf_s with the same Microsoft behavior.
int sscanf_s(const char* pBuf, const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    int numConverted = vsscanf_s(pBuf, pFormat, args);
    va_end(args);
    return numConverted;
}

#endif // _WIN32
