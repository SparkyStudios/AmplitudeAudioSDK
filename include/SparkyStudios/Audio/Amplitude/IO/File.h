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

#ifndef SPARK_AUDIO_FILE_H
#define SPARK_AUDIO_FILE_H

#include <filesystem>

#include <SparkyStudios/Audio/Amplitude/Core/Common.h>

namespace SparkyStudios::Audio::Amplitude
{
    /**
     * @brief Describes the mode in which open the file.
     */
    enum FileOpenMode
    {
        eFOM_READ = 0,
        eFOM_WRITE = 1,
        eFOM_APPEND = 2,
        eFOM_READWRITE = 3,
        eFOM_READAPPEND = 4,
    };

    /**
     * @brief The type of file being opened. This is mainly used for
     * DiskFile.
     */
    enum FileOpenKind
    {
        eFOK_BINARY = 0,
        eFOK_TEXT = 1,
    };

    /**
     * @brief Base class for a file in a FileSystem.
     */
    class AM_API_PUBLIC File
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~File() = default;

        /**
         * @brief Gets the path to the file in the loaded FileSystem.
         *
         * @return The path to the file.
         */
        [[nodiscard]] virtual AmOsString GetPath() const = 0;

        /**
         * @brief Reads a single byte from the file in an AmUInt8.
         *
         * @return The read value.
         */
        AmUInt8 Read8();

        /**
         * @brief Reads two bytes from the file in an AmUInt16.
         *
         * @return The read value.
         */
        AmUInt16 Read16();

        /**
         * @brief Reads four bytes from the file in an AmUInt32.
         *
         * @return The read value.
         */
        AmUInt32 Read32();

        /**
         * @brief Writes a single byte to the file as an AmUInt8.
         *
         * @param value The value to write.
         *
         * @return The number of byte written.
         */
        AmSize Write8(AmUInt8 value);

        /**
         * @brief Writes two bytes to the file as an AmUInt16.
         *
         * @param value The value to write.
         *
         * @return The number of byte written.
         */
        AmSize Write16(AmUInt16 value);

        /**
         * @brief Writes four bytes from the file as an AmUInt32.
         *
         * @param value The value to write.
         *
         * @return The number of byte written.
         */
        AmSize Write32(AmUInt32 value);

        /**
         * @brief Check if the read cursor is at the end of the file.
         *
         * @return True if the read cursor is at the end of the file, false otherwise.
         */
        virtual bool Eof() = 0;

        /**
         * @brief Reads data from the file.
         *
         * @param dst The destination buffer of the read data.
         * @param bytes The number of bytes to read from the file. The destination buffer must be at least as large as the number of bytes
         * to read.
         *
         * @return The number of bytes read from the file.
         */
        virtual AmSize Read(AmUInt8Buffer dst, AmSize bytes) = 0;

        /**
         * @brief Writes data to the file.
         *
         * @param src The source buffer of the data to write.
         * @param bytes The number of bytes to write to the file. The source buffer must be at least as large as the number of bytes to
         * write.
         *
         * @return The number of bytes written to the file.
         */
        virtual AmSize Write(AmConstUInt8Buffer src, AmSize bytes) = 0;

        /**
         * @brief Gets the size of the file in bytes.
         *
         * @return The size of the file in bytes.
         */
        virtual AmSize Length() = 0;

        /**
         * @brief Seeks the read/write to the specified offset.
         *
         * @param offset The offset in bytes from the beginning of the file.
         */
        void Seek(AmSize offset);

        /**
         * @brief Seeks the read/write to the specified offset, starting at the given
         * origin.
         *
         * @param offset The offset in bytes from the beginning of the file.
         * @param origin The origin from which to begin seeking.
         */
        virtual void Seek(AmSize offset, int origin) = 0;

        /**
         * @brief Gets the current position of the read/write cursor.
         *
         * @return The actual position of the read/write cursor.
         */
        virtual AmSize Position() = 0;

        /**
         * @brief Gets the pointer to the internal file handle.
         *
         * @return The internal file handle. This depends on the implementation.
         */
        virtual AmVoidPtr GetPtr();

        /**
         * @brief Checks if the file is valid.
         *
         * Validity of a file is determined by the underlying implementation. But this should
         * primarily mean that the file exists AND has been opened.
         *
         * @return True if the file is valid, false otherwise.
         */
        [[nodiscard]] virtual bool IsValid() const = 0;
    };

    /**
     * @brief A File implementation that reads and writes a file on disk.
     */
    class AM_API_PUBLIC DiskFile : public File
    {
    public:
        /**
         * @brief Creates a new DiskFile instance.
         */
        DiskFile();

        /**
         * @brief Creates a new DiskFile instance from a file handle.
         *
         * @param fp The file handle to manage in this instance.
         */
        explicit DiskFile(AmFileHandle fp);

        /**
         * @brief Creates a new DiskFile instance by opening a file at the given path.
         *
         * @param fileName The path to the file to open.
         * @param mode The open mode to use.
         * @param kind The type of file to open.
         */
        explicit DiskFile(const std::filesystem::path& fileName, FileOpenMode mode = eFOM_READ, FileOpenKind kind = eFOK_BINARY);

        /**
         * @brief Destroys the instance and release the file handler.
         */
        ~DiskFile() override;

        [[nodiscard]] AmOsString GetPath() const override;
        bool Eof() override;
        AmSize Read(AmUInt8Buffer dst, AmSize bytes) override;
        AmSize Write(AmConstUInt8Buffer src, AmSize bytes) override;
        AmSize Length() override;
        void Seek(AmSize offset, int origin) override;
        AmSize Position() override;
        AmVoidPtr GetPtr() override;
        [[nodiscard]] bool IsValid() const override;

        /**
         * @brief Opens a file at the given path.
         *
         * @param filePath The path to the file to open.
         * @param mode The open mode to use.
         * @param kind The type of file to open.
         *
         * @return The result of the operation.
         */
        AmResult Open(const std::filesystem::path& filePath, FileOpenMode mode = eFOM_READ, FileOpenKind kind = eFOK_BINARY);

        /**
         * @brief Closes the file.
         */
        void Close();

    private:
        std::filesystem::path m_filePath;
        AmFileHandle m_fileHandle;
    };

    /**
     * @brief A File implementation that reads and writes a memory buffer.
     */
    class AM_API_PUBLIC MemoryFile : public File
    {
    public:
        /**
         * @brief Creates a new MemoryFile instance.
         */
        MemoryFile();

        /**
         * @brief Creates a new MemoryFile instance from a memory buffer.
         *
         * @param buffer The memory buffer to manage in this instance.
         * @param size The size of the memory buffer.
         * @param copy If true, the memory buffer will be copied.
         * @param takeOwnership If true, the memory buffer will be owned by this instance, and released when this instance is destroyed.
         */
        MemoryFile(AmUInt8Buffer buffer, AmSize size, bool copy = false, bool takeOwnership = true);

        /**
         * @brief Destroys the instance and release the memory buffer if owned.
         */
        ~MemoryFile() override;

        [[nodiscard]] AmOsString GetPath() const override;
        bool Eof() override;
        AmSize Read(AmUInt8Buffer dst, AmSize bytes) override;
        AmSize Write(AmConstUInt8Buffer src, AmSize bytes) override;
        AmSize Length() override;
        void Seek(AmSize offset, int origin) override;
        AmSize Position() override;
        AmVoidPtr GetPtr() override;
        [[nodiscard]] bool IsValid() const override;

        /**
         * @brief Creates and open a memory file with the given size.
         *
         * @param size The size of the memory buffer.
         *
         * @return The result of the operation.
         */
        AmResult Open(AmSize size);

        /**
         * @brief Opens a memory buffer.
         *
         * @param buffer The memory buffer to open.
         * @param size The size of the memory buffer.
         * @param copy If true, the memory buffer will be copied.
         * @param takeOwnership If true, the memory buffer will be owned by this instance, and released when this instance is destroyed.
         *
         * @return The result of the operation.
         */
        AmResult OpenMem(AmConstUInt8Buffer buffer, AmSize size, bool copy = false, bool takeOwnership = true);

        /**
         * @brief Opens a memory buffer from a file content.
         *
         * @param fileName The path to the file to open.
         *
         * @return The result of the operation.
         */
        AmResult OpenToMem(const std::filesystem::path& fileName);

        /**
         * @brief Copies data from a file instance to a memory buffer. The file content is entirely copied.
         *
         * @param file The file isntance to copy data from.
         *
         * @return The result of the operation.
         */
        AmResult OpenFileToMem(File* file);

        /**
         * @brief Closes the file.
         */
        void Close();

    private:
        AmUInt8Buffer m_dataPtr;
        AmSize m_dataSize;
        AmSize m_offset;
        bool m_dataOwned;
    };
} // namespace SparkyStudios::Audio::Amplitude

#endif // SPARK_AUDIO_FILE_H
