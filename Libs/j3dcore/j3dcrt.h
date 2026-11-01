// Microsoft CRT functions for non-Windows: declared here with Microsoft's documented behavior so
// engine code calling the Windows secure CRT (sscanf_s, vsnprintf_s, strcpy_s, fopen_s, ...)
// compiles unchanged. Windows keeps the real CRT, so the whole file compiles out on _WIN32.
// See j3dcrt.c for the implementation and Tests/j3dCrtTest.c for the tests.
#ifndef J3DCRT_H
#define J3DCRT_H

#ifndef _WIN32

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <strings.h>

// Added: Microsoft typedefs (identical redefinition of an existing typedef is valid C11).
typedef size_t rsize_t;
typedef int errno_t;

#define _TRUNCATE ((size_t)-1)
#define STRUNCATE 80

#define _strcmpi strcasecmp
#define _isnan(x) isnan(x)

size_t strnlen_s(const char* pString, size_t maxsize);

errno_t strcpy_s(char* pDest, rsize_t destSize, const char* pSource);

errno_t strncpy_s(char* pDest, rsize_t destSize, const char* pSource, size_t count);

errno_t wcsncpy_s(wchar_t* pDest, rsize_t destSize, const wchar_t* pSource, size_t count);

errno_t strncat_s(char* pDest, rsize_t destSize, const char* pSource, size_t count);

int vsnprintf_s(char* pBuf, size_t bufSize, size_t count, const char* pFormat, va_list args);

int _snprintf_s(char* pBuf, size_t bufSize, size_t count, const char* pFormat, ...);

int sprintf_s(char* pBuf, size_t bufSize, const char* pFormat, ...);

int _vsnwprintf_s(wchar_t* pBuf, size_t bufSize, size_t count, const wchar_t* pFormat, va_list args);

errno_t fopen_s(FILE** ppFile, const char* pFilename, const char* pMode);

int sscanf_s(const char* pBuf, const char* pFormat, ...);

int vsscanf_s(const char* pBuf, const char* pFormat, va_list args);

#endif // _WIN32

#endif // J3DCRT_H
