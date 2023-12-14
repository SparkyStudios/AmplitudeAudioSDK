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
#include <SparkyStudios/Audio/Amplitude/IO/File.h>

namespace SparkyStudios::Audio::Amplitude
{
    AmUInt8 File::Read8()
    {
        AmUInt8 d = 0;
        Read(&d, 1);
        return d;
    }

    AmUInt16 File::Read16()
    {
        AmUInt16 d = 0;
        Read(reinterpret_cast<AmUInt8Buffer>(&d), 2);
        return d;
    }

    AmUInt32 File::Read32()
    {
        AmUInt32 d = 0;
        Read(reinterpret_cast<AmUInt8Buffer>(&d), 4);
        return d;
    }

    AmSize File::Write8(AmUInt8 value)
    {
        return Write(&value, 1);
    }

    AmSize File::Write16(AmUInt16 value)
    {
        return Write(reinterpret_cast<AmUInt8Buffer>(&value), 2);
    }

    AmSize File::Write32(AmUInt32 value)
    {
        return Write(reinterpret_cast<AmUInt8Buffer>(&value), 4);
    }

    void File::Seek(AmSize offset)
    {
        Seek(offset, SEEK_SET);
    }

    AmVoidPtr File::GetPtr()
    {
        return nullptr;
    }

    DiskFile::DiskFile()
        : DiskFile(nullptr)
    {}

    DiskFile::DiskFile(AmFileHandle fp)
        : m_fileHandle(fp)
    {}

    DiskFile::DiskFile(const std::filesystem::path& fileName, FileOpenMode mode, FileOpenKind kind)
        : DiskFile()
    {
        Open(fileName, mode, kind);
    }

    DiskFile::~DiskFile()
    {
        Close();
    }

    AmOsString DiskFile::GetPath() const
    {
        return m_filePath.c_str();
    }

    bool DiskFile::Eof()
    {
        return feof(m_fileHandle);
    }

    AmSize DiskFile::Read(AmUInt8Buffer dst, AmSize bytes)
    {
        return fread(dst, 1, bytes, m_fileHandle);
    }

    AmSize DiskFile::Write(AmConstUInt8Buffer src, AmSize bytes)
    {
        return fwrite(src, 1, bytes, m_fileHandle);
    }

    AmSize DiskFile::Length()
    {
        if (!m_fileHandle)
            return 0;

        const long pos = ftell(m_fileHandle);
        fseek(m_fileHandle, 0, SEEK_END);
        const auto len = static_cast<AmUInt32>(ftell(m_fileHandle));
        fseek(m_fileHandle, pos, SEEK_SET);

        return len;
    }

    void DiskFile::Seek(AmSize offset, int origin)
    {
        fseek(m_fileHandle, offset, origin);
    }

    AmSize DiskFile::Position()
    {
        return static_cast<AmSize>(ftell(m_fileHandle));
    }

    AmVoidPtr DiskFile::GetPtr()
    {
        return m_fileHandle;
    }

    bool DiskFile::IsValid() const
    {
        return m_fileHandle != nullptr;
    }

    AmResult DiskFile::Open(const std::filesystem::path& filePath, FileOpenMode mode, FileOpenKind kind)
    {
        if (filePath.empty())
            return AM_ERROR_INVALID_PARAMETER;

        AmOsString op{};

        switch (mode)
        {
        case eFOM_READ:
            op = kind == eFOK_TEXT ? AM_OS_STRING("r") : AM_OS_STRING("rb");
            break;
        case eFOM_WRITE:
            op = kind == eFOK_TEXT ? AM_OS_STRING("w") : AM_OS_STRING("wb");
            break;
        case eFOM_APPEND:
            op = kind == eFOK_TEXT ? AM_OS_STRING("a") : AM_OS_STRING("ab");
            break;
        case eFOM_READWRITE:
            op = kind == eFOK_TEXT ? AM_OS_STRING("w+") : AM_OS_STRING("wb+");
            break;
        case eFOM_READAPPEND:
            op = kind == eFOK_TEXT ? AM_OS_STRING("a+") : AM_OS_STRING("ab+");
            break;
        }

#if defined(AM_WINDOWS_VERSION)
        _wfopen_s(&m_fileHandle, filePath.c_str(), op.c_str());
#else
        m_fileHandle = fopen(filePath.c_str(), op.c_str());
#endif

        if (!m_fileHandle)
            return AM_ERROR_FILE_NOT_FOUND;

        m_filePath = filePath;

        return AM_ERROR_NO_ERROR;
    }

    void DiskFile::Close()
    {
        if (m_fileHandle == nullptr)
            return;

        fclose(m_fileHandle);
        m_fileHandle = nullptr;
    }

    MemoryFile::MemoryFile()
        : m_dataPtr(nullptr)
        , m_dataSize(0)
        , m_offset(0)
        , m_dataOwned(false)
    {}

    MemoryFile::MemoryFile(AmUInt8Buffer buffer, AmSize size, bool copy, bool takeOwnership)
        : MemoryFile()
    {
        OpenMem(buffer, size, copy, takeOwnership);
    }

    MemoryFile::~MemoryFile()
    {
        Close();
    }

    AmOsString MemoryFile::GetPath() const
    {
        return AM_OS_STRING("");
    }

    bool MemoryFile::Eof()
    {
        return m_offset >= m_dataSize;
    }

    AmSize MemoryFile::Read(AmUInt8Buffer dst, AmSize bytes)
    {
        if (m_offset > m_dataSize)
            return 0;

        if (m_offset + bytes >= m_dataSize)
            bytes = m_dataSize - m_offset;

        std::memcpy(dst, m_dataPtr + m_offset, bytes);
        m_offset += bytes;

        return bytes;
    }

    AmSize MemoryFile::Write(AmConstUInt8Buffer src, AmSize bytes)
    {
        const auto bytesToWrite = std::min(bytes, m_dataSize - m_offset);

        std::memcpy(m_dataPtr + m_offset, src, bytesToWrite);
        m_offset += bytesToWrite;

        return bytesToWrite;
    }

    AmSize MemoryFile::Length()
    {
        return m_dataSize;
    }

    void MemoryFile::Seek(AmSize offset, int origin)
    {
        if (origin == SEEK_SET)
            m_offset = offset;
        else if (origin == SEEK_CUR)
            m_offset += offset;
        else if (origin == SEEK_END)
            m_offset = m_dataSize + offset;
    }

    AmSize MemoryFile::Position()
    {
        return m_offset;
    }

    AmVoidPtr MemoryFile::GetPtr()
    {
        return m_dataPtr;
    }

    bool MemoryFile::IsValid() const
    {
        return m_dataPtr != nullptr;
    }

    AmResult MemoryFile::Open(AmSize size)
    {
        return OpenMem(static_cast<AmConstUInt8Buffer>(ampoolmalloc(MemoryPoolKind::IO, size)), size, false, true);
    }

    AmResult MemoryFile::OpenMem(AmConstUInt8Buffer buffer, AmSize size, bool copy, bool takeOwnership)
    {
        if (buffer == nullptr || size == 0)
            return AM_ERROR_INVALID_PARAMETER;

        if (m_dataOwned && m_dataPtr != nullptr)
            ampoolfree(MemoryPoolKind::IO, m_dataPtr);

        m_dataPtr = nullptr;
        m_offset = 0;

        m_dataSize = size;

        if (copy)
        {
            m_dataOwned = true;
            m_dataPtr = static_cast<AmUInt8Buffer>(ampoolmalloc(MemoryPoolKind::IO, size));

            if (m_dataPtr == nullptr)
                return AM_ERROR_OUT_OF_MEMORY;

            std::memcpy(m_dataPtr, buffer, size);
            return AM_ERROR_NO_ERROR;
        }

        m_dataPtr = const_cast<AmUInt8Buffer>(buffer);
        m_dataOwned = takeOwnership;

        return AM_ERROR_NO_ERROR;
    }

    AmResult MemoryFile::OpenToMem(const std::filesystem::path& fileName)
    {
        if (fileName.empty())
            return AM_ERROR_INVALID_PARAMETER;

        if (m_dataOwned && m_dataPtr != nullptr)
            ampoolfree(MemoryPoolKind::IO, m_dataPtr);

        m_dataPtr = nullptr;
        m_offset = 0;

        DiskFile df;
        if (const AmResult res = df.Open(fileName); res != AM_ERROR_NO_ERROR)
            return res;

        m_dataSize = df.Length();
        m_dataPtr = static_cast<AmUInt8Buffer>(ampoolmalloc(MemoryPoolKind::IO, m_dataSize));

        if (m_dataPtr == nullptr)
            return AM_ERROR_OUT_OF_MEMORY;

        df.Read(m_dataPtr, m_dataSize);
        m_dataOwned = true;

        df.Close();

        return AM_ERROR_NO_ERROR;
    }

    AmResult MemoryFile::OpenFileToMem(File* file)
    {
        if (!file)
            return AM_ERROR_INVALID_PARAMETER;

        if (m_dataOwned && m_dataPtr != nullptr)
            ampoolfree(MemoryPoolKind::IO, m_dataPtr);

        m_dataPtr = nullptr;
        m_offset = file->Position();

        m_dataSize = file->Length();
        m_dataPtr = static_cast<AmUInt8Buffer>(ampoolmalloc(MemoryPoolKind::IO, m_dataSize));

        if (m_dataPtr == nullptr)
            return AM_ERROR_OUT_OF_MEMORY;

        file->Seek(0, SEEK_SET);
        file->Read(m_dataPtr, m_dataSize);
        file->Seek(m_offset, SEEK_SET);

        m_dataOwned = true;

        return AM_ERROR_NO_ERROR;
    }

    void MemoryFile::Close()
    {
        if (m_dataOwned && m_dataPtr != nullptr)
            ampoolfree(MemoryPoolKind::IO, m_dataPtr);

        m_dataPtr = nullptr;
        m_dataSize = 0;
        m_offset = 0;
        m_dataOwned = false;
    }
} // namespace SparkyStudios::Audio::Amplitude
