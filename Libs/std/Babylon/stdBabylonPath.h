// Added: the Babylon column's engine-path resolver, adapter-internal. It turns an engine path
// (Windows style: backslashes, any case, relative to the working directory) into the absolute
// '/' path of the real file system entry, the way j3dcrt_ResolvePath does for the Null column.
// Unlike a lexical OS resolution, ".." is applied to the segment list before any directory is
// opened: the result differs from the operating system only across symbolic links to directories.
// The root handling assumes a POSIX absolute path ('/' as the root); the Babylon column is
// non-Windows for now, and its CMake option refuses Windows. Relative input is prefixed with
// fs.currentPath(), whose segments are trusted real directory names: the resolver never probes
// them, and each ".." that pops into them untrusts one.
#ifndef STD_STDBABYLONPATH_H
#define STD_STDBABYLONPATH_H
#ifndef __cplusplus
#error "stdBabylonPath.h is C++ only: it names babylon::files types and std::string."
#endif

#include <babylon/files/FileSystem.h>

#include <string>

// Resolves pPath through fs into out. Returns false (out unchanged) for a null or empty path,
// or when no valid path results.
bool stdBabylonPath_Resolve(babylon::files::FileSystem& fs, const char* pPath, std::string& out);

#endif // STD_STDBABYLONPATH_H
