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

#pragma once

#ifndef SPARK_AUDIO_FILESYSTEM_H
#define SPARK_AUDIO_FILESYSTEM_H

#include <filesystem>

#include <SparkyStudios/Audio/Amplitude/Core/Common.h>
#include <SparkyStudios/Audio/Amplitude/IO/File.h>

namespace SparkyStudios::Audio::Amplitude
{
    /**
     * @brief Base class for files and resources loaders.
     *
     * The FileSystem class is used by the engine as an interface to
     * load files and other resources. It provides basic functionnalities
     * needed by a filesystem
     */
    class AM_API_PUBLIC FileSystem
    {
    public:
        /**
         * @brief Default virtual destructor.
         */
        virtual ~FileSystem() = default;

        /**
         * @brief Changes the filesystem base path. That path is interpreted by the implementation
         * and doesn't necessarily have to be a real path on disk. It's just used as the base path
         * for resolving relative paths from which the engine will load resources.
         *
         * @param basePath The filesystem base path.
         */
        virtual void SetBasePath(const AmOsString& basePath) = 0;

        /**
         * @brief Gets the filesystem base path.
         *
         * @return The filesystem base path.
         */
        [[nodiscard]] virtual const AmOsString& GetBasePath() const = 0;

        /**
         * @brief Resolves a relative path from the filesystem base path.
         *
         * @return The resolved path.
         */
        [[nodiscard]] virtual AmOsString ResolvePath(const AmOsString& path) const = 0;

        /**
         * @brief Checks if an item (file or folder) exists on the filesystem.
         *
         * @param path The path to the item.
         *
         * @return True if the file exists, false otherwise.
         */
        [[nodiscard]] virtual bool Exists(const AmOsString& path) const = 0;

        /**
         * @brief Checks if an item (file or folder) is a directory.
         *
         * @param path The path to the item.
         *
         * @return True if the file is a directory, false otherwise.
         */
        [[nodiscard]] virtual bool IsDirectory(const AmOsString& path) const = 0;

        /**
         * @brief Merge the given parts of the path into a single path, by joining them with the filesystem
         * path separator.
         *
         * @param parts The parts of the path.
         *
         * @return A path concatenated with the given parts and the filesystem path separator.
         */
        [[nodiscard]] virtual AmOsString Join(const std::vector<AmOsString>& parts) const = 0;

        /**
         * @brief Opens the file at the given path.
         *
         * @param path The path to the file to open.
         *
         * @return The opened file. The returned File implementation depends on the FileSystem implementation.
         */
        [[nodiscard]] virtual std::shared_ptr<File> OpenFile(const AmOsString& path) const = 0;

        /**
         * @brief Opens the FileSystem.
         *
         * This function MUST be called before any other actions in the filesystem.
         * It is used to initialize the filesystem (eg: mounting an archive). It is
         * recommended to process the initialization in a separate thread.
         *
         * The implementation is free to ignore this if not needed.
         */
        virtual void StartOpenFileSystem() = 0;

        /**
         * @brief Checks if the FileSystem is initialized.
         *
         * Since the StartOpenFileSystem() function is designed to be asynchronous, this function
         * is used to check if the FileSystem has been successfully initialized.
         *
         * @return True if the FileSystem has been initialized, false otherwise.
         */
        virtual bool TryFinalizeOpenFileSystem() = 0;

        /**
         * @brief Stops the FileSystem.
         *
         * This function MUST be called when the FileSystem is no longer needed.
         * It is used to stop the filesystem (eg: unmounting an archive). It is
         * recommended to process the stopping in a separate thread.
         *
         * The implementation is free to ignore this if not needed.
         */
        virtual void StartCloseFileSystem() = 0;

        /**
         * @brief Checks if the FileSystem is stopped.
         *
         * Since the StartCloseFileSystem() function is designed to be asynchronous, this function
         * is used to check if the FileSystem has been successfully stopped.
         *
         * @return True if the FileSystem has been stopped, false otherwise.
         */
        virtual bool TryFinalizeCloseFileSystem() = 0;
    };

    /**
     * @brief A FileSystem implementation that reads and write files
     * from disk.
     */
    class AM_API_PUBLIC DiskFileSystem : public FileSystem
    {
    public:
        /**
         * @brief Create a new instance of the DiskFileSystem class.
         */
        DiskFileSystem();

        void SetBasePath(const AmOsString& basePath) override;
        [[nodiscard]] const AmOsString& GetBasePath() const override;
        [[nodiscard]] AmOsString ResolvePath(const AmOsString& path) const override;
        [[nodiscard]] bool Exists(const AmOsString& path) const override;
        [[nodiscard]] bool IsDirectory(const AmOsString& path) const override;
        [[nodiscard]] AmOsString Join(const std::vector<AmOsString>& parts) const override;
        [[nodiscard]] std::shared_ptr<File> OpenFile(const AmOsString& path) const override;
        void StartOpenFileSystem() override;
        bool TryFinalizeOpenFileSystem() override;
        void StartCloseFileSystem() override;
        bool TryFinalizeCloseFileSystem() override;

    private:
        std::filesystem::path _basePath;
    };

    /**
     * @brief An Amplitude resource in a FileSystem.
     *
     * This base class represents a resource (sound files, assets, etc.) in a FileSystem.
     */
    class AM_API_PUBLIC Resource
    {
    public:
        /**
         * @brief Default virtual destructor.
         */
        virtual ~Resource() = default;

        /**
         * @brief Gets the path to the resource.
         */
        [[nodiscard]] virtual const AmOsString& GetPath() const;

        /**
         * @brief Loads the resource from the given FileSystem.
         */
        virtual void Load(const FileSystem* loader) = 0;

    protected:
        AmOsString _filename;
    };
} // namespace SparkyStudios::Audio::Amplitude

#endif // SPARK_AUDIO_FILESYSTEM_H
