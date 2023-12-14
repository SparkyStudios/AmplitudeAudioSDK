// Copyright (c) 2021-present Sparky Studios. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <SparkyStudios/Audio/Amplitude/Core/Memory.h>

#include <SparkyStudios/Audio/Amplitude/IO/FileSystem.h>

namespace SparkyStudios::Audio::Amplitude
{
    DiskFileSystem::DiskFileSystem()
        : _basePath(std::filesystem::current_path())
    {}

    void DiskFileSystem::SetBasePath(const AmOsString& basePath)
    {
        const auto& p = std::filesystem::path(basePath);
        _basePath = p.is_relative() ? (std::filesystem::current_path() / p).lexically_normal().make_preferred() : p;
    }

    const AmOsString& DiskFileSystem::GetBasePath() const
    {
        return _basePath.native();
    }

    AmOsString DiskFileSystem::ResolvePath(const AmOsString& path) const
    {
        const auto& p = std::filesystem::path(path);
        return p.is_relative() ? (_basePath / p).lexically_normal().make_preferred() : p;
    }

    bool DiskFileSystem::Exists(const AmOsString& path) const
    {
        const auto& p = std::filesystem::path(ResolvePath(path));
        return exists(p);
    }

    bool DiskFileSystem::IsDirectory(const AmOsString& path) const
    {
        const auto& p = std::filesystem::path(ResolvePath(path));
        return is_directory(p);
    }

    AmOsString DiskFileSystem::Join(const std::vector<AmOsString>& parts) const
    {
        if (parts.empty())
            return AM_OS_STRING("");

        std::filesystem::path joined(parts[0]);

        for (AmSize i = 1, l = parts.size(); i < l; i++)
            joined /= parts[i];

        return joined.lexically_normal().make_preferred().native();
    }

    std::shared_ptr<File> DiskFileSystem::OpenFile(const AmOsString& path) const
    {
        auto file = std::shared_ptr<DiskFile>(ampoolnew(MemoryPoolKind::IO, DiskFile), am_delete<MemoryPoolKind::IO, DiskFile>{});
        file->Open(ResolvePath(path));

        return file;
    }

    void DiskFileSystem::StartOpenFileSystem()
    {}

    bool DiskFileSystem::TryFinalizeOpenFileSystem()
    {
        return true;
    }

    void DiskFileSystem::StartCloseFileSystem()
    {}

    bool DiskFileSystem::TryFinalizeCloseFileSystem()
    {
        return true;
    }

    const AmOsString& Resource::GetPath() const
    {
        return _filename;
    }
} // namespace SparkyStudios::Audio::Amplitude
