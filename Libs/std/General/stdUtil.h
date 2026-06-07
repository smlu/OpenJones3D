#ifndef STD_STDUTIL_H
#define STD_STDUTIL_H
#include <string.h>
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

/**
* @brief Returns the number of elements in a static array.
* @param x - The array whose length is to be calculated.
*/
#define STD_ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

/**
* @brief Macro verifies array is of type char
* @param var - The array to verify
*/
#define STD_ISSTRARRAY(var) \
       _Generic(&(var), \
        char(*)[sizeof(var)/sizeof((var)[0])]: 1, \
        const char(*)[sizeof(var)/sizeof((var)[0])]: 1, \
        default: 0)

/**
* @brief Macro verifies array is of type wchar_t
* @param var - The array to verify
*/
#define STD_ISWSTRARRAY(var) \
       _Generic(&(var), \
        wchar_t(*)[sizeof(var)/sizeof((var)[0])]: 1, \
        const char(*)[sizeof(var)/sizeof((var)[0])]: 1, \
        default: 0)

/**
 * @brief Formats a string into an array buffer.
 *
 * Ensures `buf` is an array and calls `stdUtil_Format`.
 *
 * @param buf - Array buffer for the formatted string.
 * @param format - Format string.
 * @param ... - Format arguments.
*/
#define STD_FORMAT(buf, format, ...)  \
    do {\
        _Static_assert(STD_ISSTRARRAY(buf), "STD_FORMAT requires an array type for 'buf'"); \
        stdUtil_Format(buf, STD_ARRAYLEN(buf), format, ##__VA_ARGS__); \
    } while ( 0 )

/**
 * @brief Formats a wide string into an array buffer.
 *
 * Ensures `buf` is an wchar_t array and calls `stdUtil_WFormat`.
 *
 * @param buf - wcahr_t array for the formatted string.
 * @param format - Format string.
 * @param ... - Format arguments.
*/
#define STD_WFORMAT(buf, format, ...)  \
    do {\
        _Static_assert(STD_ISWSTRARRAY(buf), "STD_WFORMAT requires an array type for 'buf'"); \
        stdUtil_WFormat(buf, STD_ARRAYLEN(buf), format, ##__VA_ARGS__); \
    } while ( 0 )

/**
 * @brief Copies a string to an array buffer.
 *
 * Ensures `dest` is an array and calls `stdUtil_StringCopy`.
 *
 * @param dest - Destination array buffer.
 * @param src - Source string.
*/
#define STD_STRCPY(dest, src)  \
    do {\
        _Static_assert(STD_ISSTRARRAY(dest), "STDCOPYSTR requires an array type for 'dest'"); \
        stdUtil_StringCopy(dest, STD_ARRAYLEN(dest), src); \
    } while ( 0 )

/**
 * @brief Copies a wide string to an array buffer.
 *
 * Ensures `dest` is an array and calls `stdUtil_WideStringCopy`.
 *
 * @param dest - Destination array buffer.
 * @param src - Source wide string.
*/
#define STD_WSTRCPY(dest, src)  \
    do {\
        _Static_assert(STD_ISWSTRARRAY(dest), "STDCOPYSTR requires an array type for 'dest'"); \
        stdUtil_WStringCopy(dest, STD_ARRAYLEN(dest), src); \
    } while ( 0 )

/**
 * @brief Copies the first num characters of source to an array buffer.
 *
 * Ensures `dest` is an array and calls `stdUtil_StringCopy`.
 *
 * @param dest - Destination array buffer.
 * @param src - Source string.
 * @param num - Number of chars to copy from `src`.
*/
#define STD_STRNCPY(dest, src, num)  \
    do {\
        _Static_assert(STD_ISSTRARRAY(dest), "STDCOPYSTR requires an array type for 'dest'"); \
        stdUtil_StringNumCopy(dest, STD_ARRAYLEN(dest), src, num); \
    } while ( 0 )

/**
 * @brief Concatenates two strings, `str1` and `str2`.
 *
 * Ensures `str1` has sufficient space to hold the concatenated result.
 * This macro wraps the functionality of `stdUtil_StringConcat`.
 *
 * @param str1 - Destination string, which will hold the result of concatenation.
 * @param str2 - Source string to append to `str1`.
*/
#define STD_STRCAT(str1, str2)  \
    do {\
        _Static_assert(STD_ISSTRARRAY(str1), "STD_STRCAT requires an array type for 'str1'"); \
        stdUtil_StringCat(str1, STD_ARRAYLEN(str1), str2); \
    } while ( 0 )

/**
 * @brief Converts wide string to string.
 *
 * Ensures `dest` is an char array and calls `stdUtil_ToAStringEx`.
 *
 * @param dest - Destination char array buffer.
 * @param src - Source wide string.
*/
#define STD_TOSTR(dest, src)  \
    do {\
        _Static_assert(STD_ISSTRARRAY(dest), "STD_TOSTR requires an array type for 'dest'"); \
        stdUtil_ToAStringEx(dest, src, STD_ARRAYLEN(dest) - 1); \
        dest[STD_ARRAYLEN(dest) - 1] = 0; \
    } while ( 0 )

/**
 * @brief Converts string to wide string.
 *
 * Ensures `dest` is an wchar_t array and calls `stdUtil_ToWStringEx`.
 *
 * @param dest - Destination whcar_t array buffer.
 * @param src - Source string.
*/
#define STD_TOWSTR(dest, src)  \
    do {\
        _Static_assert(STD_ISWSTRARRAY(dest), "STD_TOWSTR requires an array type for 'dest'"); \
        stdUtil_ToWStringEx(dest, src, STD_ARRAYLEN(dest) - 1); \
        dest[STD_ARRAYLEN(dest) - 1] = 0; \
    } while ( 0 )


/**
* Zeros out a buffer.
* @param dest - Destination data
* @param size - Size of data to zero
*/
#define STD_ZEROMEM(dest, size) memset((dest), 0, (size))

/**
* Flls a buffer with a byte value.
* @param dest - Destination data
* @param size - Size of data to fill
* @param value - Byte value to fill with
*/
#define STD_FILLMEM(dest, value, size) memset((dest), (value), (size))

/**
* Copies memory from source to destination.
* @param dest - Destination data
* @param src - Source data
* @param size - Size of data to copy
*/
#define STD_COPYMEM(dest, src, size) memcpy((dest), (src), (size))

/**
* Moves memory from source to destination.
* @param dest - Destination data
* @param src - Source data
* @param size - Size of data to move
*/
#define STD_MOVEMEM(dest, src, size) memmove((dest), (src), (size))

/**
* Compares two memory blocks.
* @param dest - First data block
* @param src - Second data block
* @param size - Size of data to compare
* @return true if equal, false otherwise
*/
#define STD_EQUALMEM(dest, src, size) (memcmp((dest), (src), (size)) == 0)


char* J3DAPI stdUtil_StringDuplicate(const char* pSource, tHostServices* pHS); // Added: From debug version
int stdUtil_Format(char* pStr, size_t size, const char* format, ...);
int stdUtil_WFormat(wchar_t* pStr, size_t size, const wchar_t* format, ...); // Added
char* J3DAPI stdUtil_StringSplit(const char* pSource, char* pFirstToken, size_t maxTokenLenght, const char* pSeparators); // Splits string on the first occurrence of pSeparators
char* J3DAPI stdUtil_ParseLiteral(const char* pSource, char* pDest, size_t destSize);

wchar_t* J3DAPI stdUtil_ToWString(const char* pString);
int J3DAPI stdUtil_ToWStringEx(wchar_t* pwString, const char* pString, size_t maxChars);

char* J3DAPI stdUtil_ToAString(const wchar_t* pwString);
void J3DAPI stdUtil_ToAStringEx(char* pString, const wchar_t* pwString, size_t maxChars);

void J3DAPI stdUtil_ToLower(char* pStr);
void J3DAPI stdUtil_ToUpper(char* pStr); // Added

int J3DAPI stdUtil_StrCmp(const char* str1, const char* str2);

bool J3DAPI stdUtil_DirExists(const char* pPath); // Added
bool J3DAPI stdUtil_FileExists(const char* pFilename); // Added

uint32_t J3DAPI stdUtil_CalcChecksum(const uint8_t* pData, size_t dataSize, uint32_t seed); // Added


/**
 * @brief Safely copies a string to a destination buffer with null-termination.
 *
 * Copies the source string `pSrc` to the destination `pDst`, ensuring the destination
 * is always null-terminated. At most `destSize - 1` characters are copied, and the copying
 * stops when a null terminator is encountered.
 *
 * @param[out] pDst - Pointer to the destination buffer.
 * @param[in] destSize - The size of the destination buffer, including the null terminator.
 * @param[in] pSrc - Pointer to the source string.
 *
 * @return `true` on success, otherwise `false`.
 */
inline bool J3DAPI stdUtil_StringCopy(char* pDest, size_t destSize, const char* pSrc)
{
    // Added: Release-build guard before calculating destSize - 1.
    STD_GUARD(pDest && pSrc && destSize, false);

    return strncpy_s(pDest, destSize, pSrc, destSize - 1) == 0;
}

/**
 * @brief Safely copies a wide string to a destination buffer with null-termination.
 *
 * Copies the source wide string `pSrc` to the destination `pDst`, ensuring the destination
 * is always null-terminated. At most `destSize - 1` characters are copied, and the copying
 * stops when a null terminator is encountered.
 *
 * @param[out] pDst - Pointer to the destination wide-character buffer.
 * @param[in] destSize - The size of the destination buffer, including the null terminator.
 * @param[in] pSrc - Pointer to the source wide string.
 *
 * @return `true` on success, otherwise `false`.
 */
inline bool J3DAPI stdUtil_WStringCopy(wchar_t* pDest, size_t destSize, const wchar_t* pSrc)
{
    // Added: Release-build guard before calculating destSize - 1.
    STD_GUARD(pDest && pSrc && destSize, false);

    return wcsncpy_s(pDest, destSize, pSrc, destSize - 1) == 0;
}

/**
 * @brief Safely copies a specified number of characters from a source string to a destination buffer.
 *
 * Copies up to `num` characters from the source string `pSrc` to the destination buffer `pDest`,
 * ensuring the destination is always null-terminated. The maximum number of characters copied is
 * the lesser of `num` and `destSize - 1`. If `num` exceeds `destSize - 1`, it is truncated.
 *
 * @param[out] pDest - Pointer to the destination buffer.
 * @param[in] destSize - The size of the destination buffer, including space for the null terminator.
 * @param[in] pSrc - Pointer to the source string.
 * @param[in] num - The maximum number of characters to copy from the source string.
 *
 * @return `true` on success, otherwise `false`.
 */
inline bool J3DAPI stdUtil_StringNumCopy(char* pDest, size_t destSize, const char* pSrc, size_t num)
{
    // Added: Release-build guard before calculating destSize - 1.
    STD_GUARD(pDest && pSrc && destSize, false);

    num = J3DMIN(num, destSize - 1);
    return strncpy_s(pDest, destSize, pSrc, num) == 0;
}

/**
 * @brief Safely copies a specified number of wide characters from a source string to a destination buffer.
 *
 * Copies up to `num` wide characters from the source string `pSrc` to the destination buffer `pDest`,
 * ensuring the destination is always null-terminated. The maximum number of characters copied is
 * the lesser of `num` and `destSize - 1`. If `num` exceeds `destSize - 1`, it is truncated.
 *
 * @param[out] pDest - Pointer to the destination wide-character buffer.
 * @param[in] destSize - The size of the destination buffer, including space for the null terminator.
 * @param[in] pSrc - Pointer to the source wide string.
 * @param[in] num - The maximum number of characters to copy from the source string.
 *
 * @return `true` on success, otherwise `false`.
 */
inline bool J3DAPI stdUtil_WStringNumCopy(wchar_t* pDest, size_t destSize, const wchar_t* pSrc, size_t num)
{
    // Added: Release-build guard before calculating destSize - 1.
    STD_GUARD(pDest && pSrc && destSize, false);

    num = J3DMIN(num, destSize - 1);
    return wcsncpy_s(pDest, destSize, pSrc, num) == 0;
}

/**
 * @brief Safely appends a string to the destination buffer with null-termination.
 *
 * Appends the source string `pSrc` to the destination `pDst`, ensuring the destination
 * buffer is always null-terminated. At most `destSize - strlen(pDst) - 1` characters are appended.
 *
 * @param[out] pDst - Pointer to the destination buffer.
 * @param[in] destSize - The total size of the destination buffer, including the null terminator.
 * @param[in] pSrc - Pointer to the source string.
 *
 * @return `true` on success, otherwise `false`.
 */
inline bool J3DAPI stdUtil_StringCat(char* pDst, size_t destSize, const char* pSrc)
{
    // Added: Release-build guard before calculating remaining capacity.
    STD_GUARD(pDst && pSrc && destSize, false);

    size_t dstLen = strnlen_s(pDst, destSize);
    if ( dstLen >= destSize )
    {
        return false;
    }
    return strncat_s(pDst, destSize, pSrc, destSize - dstLen - 1) == 0;
}

/**
 * @brief Safely appends a specified number of characters from a source string to the destination buffer.
 *
 * Appends up to `num` characters from the source string `pSrc` to the destination buffer `pDst`,
 * ensuring the destination buffer is always null-terminated. The maximum number of characters appended
 * is the lesser of `num` and the remaining space in `destSize - strlen(pDst) - 1`.
 *
 * @param[out] pDst - Pointer to the destination buffer.
 * @param[in] destSize - The total size of the destination buffer, including the null terminator.
 * @param[in] pSrc - Pointer to the source string.
 * @param[in] num - The maximum number of characters to append from the source string.
 *
 * @return `true` on success, otherwise `false`.
 */
inline bool J3DAPI stdUtil_StringNumCat(char* pDst, size_t destSize, const char* pSrc, size_t num)
{
    // Added: Release-build guard before calculating remaining capacity.
    STD_GUARD(pDst && pSrc && destSize, false);

    size_t dstLen = strnlen_s(pDst, destSize);
    if ( dstLen >= destSize )
    {
        return false;
    }
    num = J3DMIN(num, destSize - dstLen - 1);
    return strncat_s(pDst, destSize, pSrc, num) == 0;
}

// Helper hooking functions
void stdUtil_InstallHooks(void);
void stdUtil_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDUTIL_H
