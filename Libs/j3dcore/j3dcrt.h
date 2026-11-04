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
#include <stdlib.h>
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

// Added: Windows paths are case-insensitive and use backslashes. Writes the real path of pPath to
// pOut: backslashes become slashes, and each component that doesn't exist exactly is matched
// case-insensitively in its directory (first match in sorted order). Entries that differ only in
// letter case are ordered by their bytes, so the match does not depend on the order the directory
// happens to be read in. Components that match nothing, and everything after them, stay as given, so
// new files can be created. pOut is the caller's buffer of size bytes and always holds a terminated
// string: a path that does not fit, and a null pPath, yield a truncated or empty path. Returns pOut;
// when pOut is NULL or size is 0, nothing is written.
char* j3dcrt_ResolvePath(const char* pPath, char* pOut, size_t size);

// Added: fopen on the path j3dcrt_ResolvePath reports, so engine paths written with backslashes or
// in another letter case open on POSIX hosts the way they do on Windows. The FILE* it returns is the
// C runtime's, closed with fclose. Returns NULL with errno set when the file cannot be opened; a
// null pName is reported as EINVAL without touching the file system.
FILE* j3dcrt_fopen(const char* pName, const char* pMode);

// Added: Engine code keeps calling fopen; on this platform it goes through the resolver above.
// Placed after <stdio.h> so the C runtime's own declaration is not affected by the macro.
#define fopen j3dcrt_fopen

int sscanf_s(const char* pBuf, const char* pFormat, ...);

int vsscanf_s(const char* pBuf, const char* pFormat, va_list args);

// Added: Microsoft's rand and srand, the generator of the statically linked Microsoft C runtime
// the original executable uses (see j3dcrt.c): seed = seed * 214013 + 2531011, result
// (seed >> 16) & 0x7FFF. j3dcrt_rand returns the next value, 0..RAND_MAX; the state is 1 at
// program start like the CRT's (the game never calls srand) and j3dcrt_srand sets it to the seed.
// The simulation is single-threaded, so the shim keeps one process-wide state.
int j3dcrt_rand(void);

void j3dcrt_srand(unsigned int seed);

// Added: Engine code keeps calling rand and srand; on this platform they get Microsoft's generator.
// Defined only when the name is still free: a test harness that includes <j3dcore/j3d.h> through
// its forced address headers redirects rand afterwards with #undef and #define (sithVecTestAddrs.h,
// sithMultiTested.c); a redirect established before this header is processed must win over it.
// The macros are placed after <stdlib.h> so the C runtime's own declarations are not affected.
// C++ code keeps the standard names, because the macros would rewrite std::rand in the C++
// standard headers, and no C++ code in the engine calls rand.
#ifndef __cplusplus
#ifndef rand
#define rand j3dcrt_rand
#endif
#ifndef srand
#define srand j3dcrt_srand
#endif

// Added: The engine scales rand results by RAND_MAX (SITH_RAND in sithMain.h, the multiplayer
// spawn pick in sithMulti.c, the jeep bump chance in sithPhysics.c); keep Microsoft's 32767, not
// the platform CRT's. <stdlib.h> is included at the top of this header, so undef first for a
// clean redefinition.
#undef RAND_MAX
#define RAND_MAX 0x7FFF
#endif // __cplusplus

#endif // _WIN32

#endif // J3DCRT_H
