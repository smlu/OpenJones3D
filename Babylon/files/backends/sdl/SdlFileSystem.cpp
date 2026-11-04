// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/files/SdlFileSystem.h>

#include <babylon/files/Path.h>
#include <babylon/log/Log.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace babylon::files
{
namespace
{

    // '/' is the separator of the interface; the host may want another
    // one. Paths are already UTF-8, which is what SDL's file system API
    // expects on every platform.
    std::string toNativePath(std::string_view path)
    {
        std::string native(path);
#ifdef _WIN32
        std::replace(native.begin(), native.end(), '/', '\\');
#endif
        return native;
    }

    // A path SDL hands back (base path, pref path) into the interface
    // form: '/' separators and exactly one trailing '/'.
    std::string fromNativePath(std::string_view nativePath)
    {
        std::string path(nativePath);
        std::replace(path.begin(), path.end(), '\\', '/');
        if ( path.empty() || path.back() != '/' )
        {
            path.push_back('/');
        }
        return path;
    }

    // The two names of userPath(): each must be one usable path segment.
    // The same rule as in the Null backend, so both give the same answer.
    bool isSingleSegmentName(std::string_view name)
    {
        return !name.empty() && name != "." && name != ".."
            && name.find('/') == std::string_view::npos
            && name.find('\\') == std::string_view::npos
            && name.find('\0') == std::string_view::npos;
    }

    // One open file on an SDL_IOStream from SDL_IOFromFile. The stream is
    // owned here and closed in the destructor.
    class SdlFile final : public File
    {
    public:
        SdlFile(std::string path, SDL_IOStream* stream, OpenMode mode) :
            m_path(std::move(path)), m_stream(stream), m_mode(mode)
        {
        }

        ~SdlFile() override
        {
            // A failed close means buffered bytes were not written out:
            // data the caller believes saved can be lost, so it is worth
            // the one warning line.
            if ( !SDL_CloseIO(m_stream) )
            {
                log::warning("files: closing '{}' failed: {}", m_path, SDL_GetError());
            }
        }

        std::optional<std::size_t> read(std::span<std::byte> buffer) override
        {
            if ( m_mode != OpenMode::Read )
            {
                log::warning("files: read on file '{}' opened for writing", m_path);
                return std::nullopt;
            }
            if ( buffer.empty() )
            {
                return 0;   // asking for nothing succeeds at once
            }
            // SDL_ReadIO returns 0 both at the end of the stream and on
            // error; the stream status tells the two apart.
            const std::size_t num = SDL_ReadIO(m_stream, buffer.data(), buffer.size());
            if ( num < buffer.size() && SDL_GetIOStatus(m_stream) == SDL_IO_STATUS_ERROR )
            {
                log::warning("files: reading '{}' failed: {}", m_path, SDL_GetError());
                return std::nullopt;
            }
            return num;
        }

        bool write(std::span<const std::byte> data) override
        {
            if ( m_mode == OpenMode::Read )
            {
                log::warning("files: write on file '{}' opened for reading", m_path);
                return false;
            }
            if ( data.empty() )
            {
                return true;   // writing nothing succeeds at once
            }
            const std::size_t num = SDL_WriteIO(m_stream, data.data(), data.size());
            if ( num != data.size() )
            {
                log::warning("files: writing '{}' failed: wrote {} of {} bytes: {}",
                    m_path, num, data.size(), SDL_GetError());
                return false;
            }
            return true;
        }

        bool seek(std::int64_t offset, SeekOrigin origin) override
        {
            std::int64_t base = 0;
            if ( origin == SeekOrigin::Current )
            {
                const Sint64 current = SDL_TellIO(m_stream);
                if ( current < 0 )
                {
                    log::warning("files: seek on '{}' failed: {}", m_path, SDL_GetError());
                    return false;
                }
                base = current;
            }
            else if ( origin == SeekOrigin::End )
            {
                const Sint64 streamSize = SDL_GetIOSize(m_stream);
                if ( streamSize < 0 )
                {
                    log::warning("files: seek on '{}' failed: {}", m_path, SDL_GetError());
                    return false;
                }
                base = streamSize;
            }

            const std::int64_t target = base + offset;
            if ( target < 0 )
            {
                log::warning("files: seek to {} on file '{}' lands before the beginning",
                    target, m_path);
                return false;
            }
            if ( m_mode == OpenMode::Read )
            {
                // A file opened for reading has nothing past its end and
                // the host would let the seek through into a failing read;
                // the interface refuses it here instead, before the seek.
                const Sint64 streamSize = SDL_GetIOSize(m_stream);
                if ( streamSize < 0 )
                {
                    log::warning("files: seek on '{}' failed: {}", m_path, SDL_GetError());
                    return false;
                }
                if ( target > streamSize )
                {
                    log::warning("files: can not seek past the end of file '{}' "
                        "opened for reading", m_path);
                    return false;
                }
            }
            if ( SDL_SeekIO(m_stream, target, SDL_IO_SEEK_SET) < 0 )
            {
                log::warning("files: seek on '{}' failed: {}", m_path, SDL_GetError());
                return false;
            }
            return true;
        }

        std::optional<std::int64_t> tell() const override
        {
            const Sint64 current = SDL_TellIO(m_stream);
            if ( current < 0 )
            {
                log::warning("files: telling the position in '{}' failed: {}",
                    m_path, SDL_GetError());
                return std::nullopt;
            }
            return current;
        }

        std::optional<std::int64_t> size() const override
        {
            const Sint64 streamSize = SDL_GetIOSize(m_stream);
            if ( streamSize < 0 )
            {
                log::warning("files: asking the size of '{}' failed: {}",
                    m_path, SDL_GetError());
                return std::nullopt;
            }
            return streamSize;
        }

    private:
        std::string m_path;   // as given to open(), for warnings
        SDL_IOStream* m_stream;
        OpenMode m_mode;
    };

    // One enumerated entry: SDL hands the bare name, so the type comes
    // from a stat of the full path. An entry that cannot be statted (a
    // broken symbolic link, or one removed during the listing) is reported
    // as a file rather than failing the whole listing.
    SDL_EnumerationResult SDLCALL collectEntry(void* userdata, const char* dirname,
        const char* fname)
    {
        auto* const entries = static_cast<std::vector<DirectoryEntry>*>(userdata);
        const std::string_view name(fname);
        if ( name == "." || name == ".." )
        {
            return SDL_ENUM_CONTINUE;   // SDL3 already skips them; the interface promises it
        }
        std::string full(dirname);   // dirname ends with the native separator
        full.append(fname);
        SDL_PathInfo info;
        const bool isDirectory =
            SDL_GetPathInfo(full.c_str(), &info) && info.type == SDL_PATHTYPE_DIRECTORY;
        if ( !isDirectory )
        {
            SDL_ClearError();
        }
        entries->push_back({std::string(name), isDirectory});
        return SDL_ENUM_CONTINUE;
    }

    class SdlFileSystem final : public FileSystem
    {
    public:
        std::unique_ptr<File> open(std::string_view path, OpenMode mode) override
        {
            if ( !isValidPath(path) )
            {
                log::warning("files: rejected invalid path '{}'", path);
                return nullptr;
            }
            // Opening a directory behaves per host, so the interface rule
            // is checked here first: a directory is never a file.
            const std::string native = toNativePath(path);
            SDL_PathInfo info;
            if ( SDL_GetPathInfo(native.c_str(), &info) && info.type == SDL_PATHTYPE_DIRECTORY )
            {
                log::warning("files: can not open directory '{}' as a file", path);
                return nullptr;
            }
            const char* const modeString =
                ( mode == OpenMode::Read ) ? "rb" : ( mode == OpenMode::Write ) ? "wb" : "ab";
            SDL_IOStream* const stream = SDL_IOFromFile(native.c_str(), modeString);
            if ( stream == nullptr )
            {
                log::warning("files: opening '{}' failed: {}", path, SDL_GetError());
                return nullptr;
            }
            return std::make_unique<SdlFile>(std::string(path), stream, mode);
        }

        bool exists(std::string_view path) override
        {
            if ( !isValidPath(path) )
            {
                return false;   // the interface promises no logging here
            }
            SDL_PathInfo info;
            const std::string native = toNativePath(path);
            if ( !SDL_GetPathInfo(native.c_str(), &info) )
            {
                // On this backend a failed stat also means "does not
                // exist"; exists() answers, it does not fail.
                SDL_ClearError();
                return false;
            }
            return info.type == SDL_PATHTYPE_FILE || info.type == SDL_PATHTYPE_DIRECTORY;
        }

        bool createDirectory(std::string_view path) override
        {
            if ( !isValidPath(path) )
            {
                log::warning("files: rejected invalid path '{}'", path);
                return false;
            }
            // SDL_CreateDirectory makes missing parents and reports success
            // when the directory already exists, which is the promise of
            // this call.
            const std::string native = toNativePath(path);
            if ( !SDL_CreateDirectory(native.c_str()) )
            {
                log::warning("files: creating directory '{}' failed: {}", path, SDL_GetError());
                return false;
            }
            return true;
        }

        bool remove(std::string_view path) override
        {
            if ( !isValidPath(path) )
            {
                log::warning("files: rejected invalid path '{}'", path);
                return false;
            }
            const std::string native = toNativePath(path);
            if ( SDL_RemovePath(native.c_str()) )
            {
                return true;   // deleted, or never there: both mean gone
            }
            const std::string error = SDL_GetError();
            SDL_PathInfo info;
            if ( !SDL_GetPathInfo(native.c_str(), &info) || info.type == SDL_PATHTYPE_NONE )
            {
                SDL_ClearError();
                return true;   // the promise holds anyway: gone afterwards
            }
            log::warning("files: removing '{}' failed: {}", path, error);
            return false;
        }

        std::optional<std::vector<DirectoryEntry>> listDirectory(std::string_view path) override
        {
            if ( !isValidPath(path) )
            {
                log::warning("files: rejected invalid path '{}'", path);
                return std::nullopt;
            }
            const std::string native = toNativePath(path);
            SDL_PathInfo info;
            if ( !SDL_GetPathInfo(native.c_str(), &info) || info.type != SDL_PATHTYPE_DIRECTORY )
            {
                SDL_ClearError();
                log::warning("files: can not list '{}': not a readable directory", path);
                return std::nullopt;
            }

            std::vector<DirectoryEntry> entries;
            if ( !SDL_EnumerateDirectory(native.c_str(), collectEntry, &entries) )
            {
                log::warning("files: listing '{}' failed: {}", path, SDL_GetError());
                return std::nullopt;
            }
            std::sort(entries.begin(), entries.end(),
                [](const DirectoryEntry& a, const DirectoryEntry& b) { return a.name < b.name; });
            return entries;
        }

        std::string basePath() override
        {
            // SDL caches this string and owns it; it must not be freed.
            const char* const base = SDL_GetBasePath();
            if ( base == nullptr )
            {
                log::warning("files: finding the base path failed: {}", SDL_GetError());
                return {};
            }
            return fromNativePath(base);
        }

        std::string currentPath() override
        {
            // SDL hands the working directory to the caller, owned here,
            // with native separators and a trailing separator (SDL 3.2+).
            char* const directory = SDL_GetCurrentDirectory();
            if ( directory == nullptr )
            {
                log::warning("files: finding the current directory failed: {}", SDL_GetError());
                return {};
            }
            std::string result = fromNativePath(directory);
            SDL_free(directory);
            if ( !isValidPath(result) )
            {
                log::warning("files: the current path '{}' is not usable", result);
                return {};
            }
            return result;
        }

        std::string userPath(std::string_view organization,
            std::string_view application) override
        {
            if ( !isSingleSegmentName(organization) || !isSingleSegmentName(application) )
            {
                log::warning("files: can not make a user path from '{}' and '{}'",
                    organization, application);
                return {};
            }
            const std::string org(organization);
            const std::string app(application);
            // SDL_GetPrefPath creates the directory, which the interface
            // promises, and returns it with native separators, owned by
            // the caller (ADR-0019).
            char* const pref = SDL_GetPrefPath(org.c_str(), app.c_str());
            if ( pref == nullptr )
            {
                log::warning("files: creating the user path for '{}' / '{}' failed: {}",
                    org, app, SDL_GetError());
                return {};
            }
            std::string result = fromNativePath(pref);
            SDL_free(pref);
            if ( !isValidPath(result) )
            {
                log::warning("files: the user path '{}' is not usable", result);
                return {};
            }
            return result;
        }
    };

}

    std::unique_ptr<FileSystem> createSdlFileSystem()
    {
        return std::make_unique<SdlFileSystem>();
    }

}
