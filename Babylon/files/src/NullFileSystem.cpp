// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/files/NullFileSystem.h>

#include <babylon/files/Path.h>
#include <babylon/log/Log.h>

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

    // The absolute path of a valid path: rooted at '/', without a trailing
    // '/', the root itself being "/". Relative paths resolve against the
    // root, so "a/b" becomes "/a/b" and "a/" becomes "/a".
    std::string absolutePath(std::string_view path)
    {
        std::string result;
        result.reserve(path.size() + 1);
        if ( path.front() != '/' )
        {
            result.push_back('/');
        }
        result.append(path);
        if ( result.size() > 1 && result.back() == '/' )
        {
            result.pop_back();
        }
        return result;
    }

    // The directory key that holds the absolute path "/a/b": "/a/". The
    // root is its own parent for the walk here, which never asks.
    std::string parentKey(std::string_view absolute)
    {
        const std::size_t slash = absolute.rfind('/');
        if ( slash == std::string::npos || slash == 0 )
        {
            return "/";
        }
        return std::string(absolute.substr(0, slash + 1));
    }

    // The directory key of an absolute path: the path plus a trailing '/'.
    std::string directoryKey(std::string_view absolute)
    {
        if ( absolute == "/" )
        {
            return "/";
        }
        std::string key(absolute);
        key.push_back('/');
        return key;
    }

    // The name part of an absolute path or directory key, trailing '/'
    // ignored.
    std::string baseName(std::string_view path)
    {
        std::string_view name = path;
        if ( name.size() > 1 && name.back() == '/' )
        {
            name.remove_suffix(1);
        }
        const std::size_t slash = name.rfind('/');
        return std::string(name.substr(slash == std::string_view::npos ? 0 : slash + 1));
    }

    // One open Null file. m_data is shared with the file system and with
    // every other handle of the same path; the position is per handle.
    class NullFile final : public File
    {
    public:
        NullFile(std::string path, OpenMode mode, std::shared_ptr<std::vector<std::byte>> data) :
            m_path(std::move(path)), m_mode(mode), m_data(std::move(data)),
            m_position(mode == OpenMode::Append ? static_cast<std::int64_t>(m_data->size()) : 0)
        {
        }

        std::optional<std::size_t> read(std::span<std::byte> buffer) override
        {
            if ( m_mode != OpenMode::Read )
            {
                log::warning("files: read on file '{}' opened for writing", m_path);
                return std::nullopt;
            }
            const std::int64_t fileSize = static_cast<std::int64_t>(m_data->size());
            if ( m_position >= fileSize )
            {
                return 0;   // the end of the file, not an error
            }
            const std::size_t num = std::min<std::size_t>(
                buffer.size(), static_cast<std::size_t>(fileSize - m_position));
            std::copy(m_data->begin() + static_cast<std::ptrdiff_t>(m_position),
                m_data->begin() + static_cast<std::ptrdiff_t>(m_position) + static_cast<std::ptrdiff_t>(num),
                buffer.begin());
            m_position += static_cast<std::int64_t>(num);
            return num;
        }

        bool write(std::span<const std::byte> data) override
        {
            if ( m_mode == OpenMode::Read )
            {
                log::warning("files: write on file '{}' opened for reading", m_path);
                return false;
            }
            std::vector<std::byte>& contents = *m_data;
            if ( m_mode == OpenMode::Append )
            {
                m_position = static_cast<std::int64_t>(contents.size());
            }
            const std::size_t start = static_cast<std::size_t>(m_position);
            if ( contents.size() < start + data.size() )
            {
                // A write past the end fills the gap with zero bytes, the
                // way a host file system extends a file.
                contents.resize(start + data.size());
            }
            std::copy(data.begin(), data.end(),
                contents.begin() + static_cast<std::ptrdiff_t>(start));
            m_position = static_cast<std::int64_t>(start + data.size());
            return true;
        }

        bool seek(std::int64_t offset, SeekOrigin origin) override
        {
            std::int64_t target = offset;
            if ( origin == SeekOrigin::Current )
            {
                target += m_position;
            }
            else if ( origin == SeekOrigin::End )
            {
                target += static_cast<std::int64_t>(m_data->size());
            }
            if ( target < 0 )
            {
                log::warning("files: seek to {} on file '{}' lands before the beginning",
                    target, m_path);
                return false;
            }
            if ( m_mode == OpenMode::Read &&
                target > static_cast<std::int64_t>(m_data->size()) )
            {
                log::warning("files: can not seek past the end of file '{}' opened for reading",
                    m_path);
                return false;
            }
            m_position = target;
            return true;
        }

        std::optional<std::int64_t> tell() const override
        {
            return m_position;
        }

        std::optional<std::int64_t> size() const override
        {
            return static_cast<std::int64_t>(m_data->size());
        }

    private:
        std::string m_path;   // as given to open(), for warnings
        OpenMode m_mode;
        std::shared_ptr<std::vector<std::byte>> m_data;
        std::int64_t m_position;
    };

    // The two names of userPath(): each must be one usable path segment.
    bool isSingleSegmentName(std::string_view name)
    {
        return !name.empty() && name != "." && name != ".."
            && name.find('/') == std::string_view::npos
            && name.find('\\') == std::string_view::npos
            && name.find('\0') == std::string_view::npos;
    }

}

    NullFileSystem::NullFileSystem()
    {
        m_directories.insert("/");
        m_directories.insert("/base/");
    }

    std::unique_ptr<File> NullFileSystem::open(std::string_view path, OpenMode mode)
    {
        if ( !isValidPath(path) )
        {
            log::warning("files: rejected invalid path '{}'", path);
            return nullptr;
        }
        const std::string absolute = absolutePath(path);
        if ( m_directories.find(directoryKey(absolute)) != m_directories.end() )
        {
            log::warning("files: can not open directory '{}' as a file", path);
            return nullptr;
        }
        if ( mode != OpenMode::Read &&
            m_directories.find(parentKey(absolute)) == m_directories.end() )
        {
            log::warning("files: can not open '{}': its parent directory does not exist", path);
            return nullptr;
        }

        const auto existing = m_files.find(absolute);
        std::shared_ptr<std::vector<std::byte>> data;
        if ( mode == OpenMode::Read )
        {
            if ( existing == m_files.end() )
            {
                log::warning("files: can not open missing file '{}'", path);
                return nullptr;
            }
            data = existing->second;
        }
        else if ( mode == OpenMode::Write )
        {
            if ( existing != m_files.end() )
            {
                // Truncate in place, as creating a file with "wb" does on
                // a host: handles that already have the file open see it.
                existing->second->clear();
                data = existing->second;
            }
            else
            {
                data = std::make_shared<std::vector<std::byte>>();
                m_files.emplace(absolute, data);
            }
        }
        else   // Append
        {
            if ( existing != m_files.end() )
            {
                data = existing->second;
            }
            else
            {
                data = std::make_shared<std::vector<std::byte>>();
                m_files.emplace(absolute, data);
            }
        }
        return std::make_unique<NullFile>(std::string(path), mode, std::move(data));
    }

    bool NullFileSystem::exists(std::string_view path)
    {
        // The interface promises no logging here: probing paths that need
        // not exist is a normal search, not a failure.
        if ( !isValidPath(path) )
        {
            return false;
        }
        const std::string absolute = absolutePath(path);
        if ( path.back() == '/' )
        {
            return m_directories.find(directoryKey(absolute)) != m_directories.end();
        }
        return m_files.find(absolute) != m_files.end()
            || m_directories.find(directoryKey(absolute)) != m_directories.end();
    }

    bool NullFileSystem::createDirectory(std::string_view path)
    {
        if ( !isValidPath(path) )
        {
            log::warning("files: rejected invalid path '{}'", path);
            return false;
        }
        const std::string absolute = absolutePath(path);
        if ( m_directories.find(directoryKey(absolute)) != m_directories.end() )
        {
            return true;   // already a directory, the promise holds
        }

        // Walk from the root down. A file standing where a parent or the
        // path itself must become a directory fails the whole request, as
        // mkdir -p does on a host.
        std::string current;
        for ( std::size_t i = 1; i < absolute.size(); )
        {
            std::size_t slash = absolute.find('/', i);
            if ( slash == std::string::npos )
            {
                slash = absolute.size();
            }
            current.push_back('/');
            current.append(absolute.substr(i, slash - i));
            if ( m_files.find(current) != m_files.end() )
            {
                log::warning("files: can not create directory '{}': '{}' is a file",
                    path, current);
                return false;
            }
            m_directories.insert(current + "/");
            i = slash + 1;
        }
        return true;
    }

    bool NullFileSystem::remove(std::string_view path)
    {
        if ( !isValidPath(path) )
        {
            log::warning("files: rejected invalid path '{}'", path);
            return false;
        }
        const std::string absolute = absolutePath(path);
        if ( absolute == "/" )
        {
            log::warning("files: the root directory of a NullFileSystem can not be removed");
            return false;
        }

        const auto file = m_files.find(absolute);
        if ( file != m_files.end() )
        {
            if ( file->second.use_count() > 1 )
            {
                log::warning("files: can not remove '{}': the file is still open", path);
                return false;
            }
            m_files.erase(file);
            return true;
        }

        const std::string key = directoryKey(absolute);
        if ( m_directories.find(key) != m_directories.end() )
        {
            const bool hasFiles = std::any_of(m_files.begin(), m_files.end(),
                [&key](const auto& entry) { return parentKey(entry.first) == key; });
            const bool hasChildren = std::any_of(m_directories.begin(), m_directories.end(),
                [&key](const std::string& otherKey)
                {
                    if ( otherKey == key || otherKey == "/" )
                    {
                        return false;
                    }
                    return parentKey(otherKey.substr(0, otherKey.size() - 1)) == key;
                });
            if ( hasFiles || hasChildren )
            {
                log::warning("files: can not remove directory '{}': it is not empty", path);
                return false;
            }
            m_directories.erase(key);
            return true;
        }
        return true;   // never there: gone afterwards, and not a failure
    }

    std::optional<std::vector<DirectoryEntry>> NullFileSystem::listDirectory(std::string_view path)
    {
        if ( !isValidPath(path) )
        {
            log::warning("files: rejected invalid path '{}'", path);
            return std::nullopt;
        }
        const std::string key = directoryKey(absolutePath(path));
        if ( m_directories.find(key) == m_directories.end() )
        {
            log::warning("files: can not list '{}': not a readable directory", path);
            return std::nullopt;
        }

        std::vector<DirectoryEntry> entries;
        for ( const auto& entry : m_files )
        {
            if ( parentKey(entry.first) == key )
            {
                entries.push_back({baseName(entry.first), false});
            }
        }
        for ( const std::string& otherKey : m_directories )
        {
            if ( otherKey == key || otherKey == "/" )
            {
                continue;
            }
            const std::string canonical = otherKey.substr(0, otherKey.size() - 1);
            if ( parentKey(canonical) == key )
            {
                entries.push_back({baseName(canonical), true});
            }
        }
        std::sort(entries.begin(), entries.end(),
            [](const DirectoryEntry& a, const DirectoryEntry& b) { return a.name < b.name; });
        return entries;
    }

    std::optional<std::vector<std::string>> NullFileSystem::listNames(std::string_view path)
    {
        if ( !isValidPath(path) )
        {
            log::warning("files: rejected invalid path '{}'", path);
            return std::nullopt;
        }
        const std::string key = directoryKey(absolutePath(path));
        if ( m_directories.find(key) == m_directories.end() )
        {
            log::warning("files: can not list '{}': not a readable directory", path);
            return std::nullopt;
        }

        // The same names as listDirectory(), gathered without building a
        // DirectoryEntry per child: the tree's keys already answer what this
        // call reports, so no metadata is read at all.
        std::vector<std::string> names;
        for ( const auto& entry : m_files )
        {
            if ( parentKey(entry.first) == key )
            {
                names.push_back(baseName(entry.first));
            }
        }
        for ( const std::string& otherKey : m_directories )
        {
            if ( otherKey == key || otherKey == "/" )
            {
                continue;
            }
            const std::string canonical = otherKey.substr(0, otherKey.size() - 1);
            if ( parentKey(canonical) == key )
            {
                names.push_back(baseName(canonical));
            }
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    std::string NullFileSystem::basePath()
    {
        return "/base/";
    }

    std::string NullFileSystem::currentPath()
    {
        return "/";   // the tree's root is where relative paths resolve
    }

    std::string NullFileSystem::userPath(std::string_view organization,
        std::string_view application)
    {
        if ( !isSingleSegmentName(organization) || !isSingleSegmentName(application) )
        {
            log::warning("files: can not make a user path from '{}' and '{}'",
                organization, application);
            return {};
        }
        std::string path = "/user/";
        path.append(organization);
        path.push_back('/');
        path.append(application);
        path.push_back('/');
        if ( !isValidPath(path) )
        {
            log::warning("files: the user path for '{}' and '{}' is too long",
                organization, application);
            return {};
        }
        if ( !createDirectory(path) )
        {
            return {};   // createDirectory logged the one warning
        }
        return path;
    }

    bool NullFileSystem::addFile(std::string_view path, std::span<const std::byte> contents)
    {
        if ( !isValidPath(path) )
        {
            log::warning("files: rejected invalid path '{}'", path);
            return false;
        }
        const std::string absolute = absolutePath(path);
        if ( absolute == "/" || m_directories.find(directoryKey(absolute)) != m_directories.end() )
        {
            log::warning("files: a directory exists at '{}'", path);
            return false;
        }
        const std::string parent = parentKey(absolute);
        if ( m_directories.find(parent) == m_directories.end() && !createDirectory(parent) )
        {
            return false;   // createDirectory logged the one warning
        }

        const auto existing = m_files.find(absolute);
        if ( existing != m_files.end() )
        {
            // Replace in place: handles that have the file open see it.
            existing->second->assign(contents.begin(), contents.end());
        }
        else
        {
            m_files.emplace(absolute,
                std::make_shared<std::vector<std::byte>>(contents.begin(), contents.end()));
        }
        return true;
    }

}
